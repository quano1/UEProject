// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_RigHierarchyDefines.h"
#include "TLLRN_RigHierarchyMetadata.h"
#include "TLLRN_TLLRN_RigConnectionRules.h"
#include "TLLRN_RigHierarchyElements.generated.h"

struct FRigVMExecuteContext;
struct FTLLRN_RigBaseElement;
struct FTLLRN_RigControlElement;
class UTLLRN_RigHierarchy;
class FTLLRN_RigElementKeyRedirector;

DECLARE_DELEGATE_RetVal_ThreeParams(FTransform, FRigReferenceGetWorldTransformDelegate, const FRigVMExecuteContext*, const FTLLRN_RigElementKey& /* Key */, bool /* bInitial */);
DECLARE_DELEGATE_TwoParams(FTLLRN_RigElementMetadataChangedDelegate, const FTLLRN_RigElementKey& /* Key */, const FName& /* Name */);
DECLARE_DELEGATE_ThreeParams(FTLLRN_RigElementMetadataTagChangedDelegate, const FTLLRN_RigElementKey& /* Key */, const FName& /* Tag */, bool /* AddedOrRemoved */);
DECLARE_DELEGATE_RetVal_ThreeParams(FTransform, FRigReferenceGetWorldTransformDelegate, const FRigVMExecuteContext*, const FTLLRN_RigElementKey& /* Key */, bool /* bInitial */);

#define DECLARE_RIG_ELEMENT_METHODS(ElementType) \
template<typename T> \
friend const T* Cast(const ElementType* InElement) \
{ \
   return Cast<T>((const FTLLRN_RigBaseElement*) InElement); \
} \
template<typename T> \
friend T* Cast(ElementType* InElement) \
{ \
   return Cast<T>((FTLLRN_RigBaseElement*) InElement); \
} \
template<typename T> \
friend const T* CastChecked(const ElementType* InElement) \
{ \
	return CastChecked<T>((const FTLLRN_RigBaseElement*) InElement); \
} \
template<typename T> \
friend T* CastChecked(ElementType* InElement) \
{ \
	return CastChecked<T>((FTLLRN_RigBaseElement*) InElement); \
} \
virtual int32 GetElementTypeIndex() const override { return (int32)ElementType::ElementTypeIndex; }

UENUM()
namespace ETLLRN_RigTransformType
{
	enum Type : int
	{
		InitialLocal,
		CurrentLocal,
		InitialGlobal,
		CurrentGlobal,
		NumTransformTypes
	};
}

namespace ETLLRN_RigTransformType
{
	inline ETLLRN_RigTransformType::Type SwapCurrentAndInitial(const Type InTransformType)
	{
		switch(InTransformType)
		{
			case CurrentLocal:
			{
				return InitialLocal;
			}
			case CurrentGlobal:
			{
				return InitialGlobal;
			}
			case InitialLocal:
			{
				return CurrentLocal;
			}
			default:
			{
				break;
			}
		}
		return CurrentGlobal;
	}

	inline Type SwapLocalAndGlobal(const Type InTransformType)
	{
		switch(InTransformType)
		{
			case CurrentLocal:
			{
				return CurrentGlobal;
			}
			case CurrentGlobal:
			{
				return CurrentLocal;
			}
			case InitialLocal:
			{
				return InitialGlobal;
			}
			default:
			{
				break;
			}
		}
		return InitialLocal;
	}

	inline Type MakeLocal(const Type InTransformType)
	{
		switch(InTransformType)
		{
			case CurrentLocal:
			case CurrentGlobal:
			{
				return CurrentLocal;
			}
			default:
			{
				break;
			}
		}
		return InitialLocal;
	}

	inline Type MakeGlobal(const Type InTransformType)
	{
		switch(InTransformType)
		{
			case CurrentLocal:
			case CurrentGlobal:
			{
				return CurrentGlobal;
			}
			default:
			{
				break;
			}
		}
		return InitialGlobal;
	}

	inline Type MakeInitial(const Type InTransformType)
	{
		switch(InTransformType)
		{
			case CurrentLocal:
			case InitialLocal:
			{
				return InitialLocal;
			}
			default:
			{
				break;
			}
		}
		return InitialGlobal;
	}

	inline Type MakeCurrent(const Type InTransformType)
	{
		switch(InTransformType)
		{
			case CurrentLocal:
        	case InitialLocal:
			{
				return CurrentLocal;
			}
			default:
			{
				break;
			}
		}
		return CurrentGlobal;
	}

	inline bool IsLocal(const Type InTransformType)
	{
		switch(InTransformType)
		{
			case CurrentLocal:
        	case InitialLocal:
			{
				return true;
			}
			default:
			{
				break;
			}
		}
		return false;
	}

	inline bool IsGlobal(const Type InTransformType)
	{
		switch(InTransformType)
		{
			case CurrentGlobal:
        	case InitialGlobal:
			{
				return true;;
			}
			default:
			{
				break;
			}
		}
		return false;
	}

	inline bool IsInitial(const Type InTransformType)
	{
		switch(InTransformType)
		{
			case InitialLocal:
        	case InitialGlobal:
			{
				return true;
			}
			default:
			{
				break;
			}
		}
		return false;
	}

	inline bool IsCurrent(const Type InTransformType)
	{
		switch(InTransformType)
		{
			case CurrentLocal:
        	case CurrentGlobal:
			{
				return true;
			}
			default:
			{
				break;
			}
		}
		return false;
	}
}

UENUM()
namespace ETLLRN_RigTransformStorageType
{
	enum Type : int
	{
		Pose,
		Offset,
		Shape,
		NumStorageTypes
	};
}

template<typename T>
struct TLLRN_CONTROLRIG_API FRigReusableElementStorage
{
	TArray<T> Storage;
	TArray<int32> FreeList;

	bool IsValidIndex(const int32& InIndex) const
	{
		return Storage.IsValidIndex(InIndex);
	}

	int32 Num() const
	{
		return Storage.Num();
	}

	T& operator[](int InIndex)
	{
		return Storage[InIndex];
	}

	const T& operator[](int InIndex) const
	{
		return Storage[InIndex];
	}

	typename TArray<T>::RangedForIteratorType begin() { return Storage.begin(); }
	typename TArray<T>::RangedForIteratorType end() { return Storage.end(); }
	typename TArray<T>::RangedForConstIteratorType begin() const { return Storage.begin(); }
	typename TArray<T>::RangedForConstIteratorType end() const { return Storage.end(); }

	const T* GetData() const
	{
		return Storage.GetData();
	}

	TArray<int32, TInlineAllocator<4>> Allocate(int32 InCount, const T& InDefault)
	{
		TArray<int32, TInlineAllocator<4>> Indices;
	
		const int32 NumToAllocate = InCount - FMath::Min(InCount, FreeList.Num());
		if(NumToAllocate > 0)
		{
			Storage.Reserve(Storage.Num() + NumToAllocate);
		}
	
		for(int32 Index = 0; Index < InCount; Index++)
		{
			if(FreeList.IsEmpty())
			{
				Indices.Push(Storage.Add(InDefault));
			}
			else
			{
				Indices.Push(FreeList.Pop(EAllowShrinking::No));
				Storage[Indices.Last()] = InDefault;
			}
		}

		return Indices;
	}

	void Deallocate(int32& InIndex, T** InStorage)
	{
		if(InIndex == INDEX_NONE)
		{
			return;
		}
#if WITH_EDITOR
		check(Storage.IsValidIndex(InIndex));
		check(!FreeList.Contains(InIndex));
#endif
		FreeList.Add(InIndex);
		InIndex = INDEX_NONE;
		if(InStorage)
		{
			*InStorage = nullptr;
		}
	}

	void Deallocate(const TConstArrayView<int32>& InIndices)
	{
		if(InIndices.IsEmpty())
		{
			return;
		}
		FreeList.Reserve(FreeList.Num() + InIndices.Num());
		for(int32 Index : InIndices)
		{
			if(Index != INDEX_NONE && !FreeList.Contains(Index))
			{
				Deallocate(Index, nullptr);
			}
		}
	}

	template<typename OwnerType>
	void Deallocate(OwnerType* InOwner)
	{
		check(InOwner);
		Deallocate(InOwner->StorageIndex, &InOwner->Storage);
	}

	void Reset(TFunction<void(int32, T&)> OnDestroyCallback = nullptr)
	{
		if(OnDestroyCallback)
		{
			for(int32 Index = 0; Index < Storage.Num(); Index++)
			{
				OnDestroyCallback(Index, Storage[Index]);
			}
		}
		Storage.Reset();
		FreeList.Reset();
	}

	bool Contains(int32 InIndex, const T* InStorage)
	{
		if(!IsValidIndex(InIndex))
		{
			return false;
		}
		return GetData() + InIndex == InStorage;
	}

	template<typename OwnerType>
	bool Contains(const OwnerType* InOwner)
	{
		check(InOwner);
		return Contains(InOwner->StorageIndex, InOwner->Storage);
	}

	TMap<int32, int32> Shrink(TFunction<void(int32, T&)> OnDestroyCallback = nullptr)
	{
		TMap<int32, int32> OldToNew;
		
		if(!FreeList.IsEmpty())
		{
			TArray<bool> ToRemove;

			if(FreeList.Num() != Storage.Num() || OnDestroyCallback != nullptr)
			{
				ToRemove.AddZeroed(Storage.Num());
				for(int32 FreeIndex : FreeList)
				{
					ToRemove[FreeIndex] = true;
					if(OnDestroyCallback)
					{
						OnDestroyCallback(FreeIndex, Storage[FreeIndex]);
					}
				}
			}

			if(FreeList.Num() != Storage.Num())
			{
				TArray<T> NewStorage;
				NewStorage.Reserve(FMath::Max(Storage.Num() - FreeList.Num(), 0));
				for(int32 OldIndex = 0; OldIndex < Storage.Num(); OldIndex++)
				{
					if(!ToRemove[OldIndex])
					{
						OldToNew.Add(OldIndex, NewStorage.Add(Storage[OldIndex]));
					}
				}
				Storage = NewStorage;
			}
			else
			{
				Storage.Reset();
			}

			FreeList.Reset();
		}

		FreeList.Shrink();
		Storage.Shrink();

		return OldToNew;
	}
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigTransformDirtyState
{
public:

	GENERATED_BODY()

	FTLLRN_RigTransformDirtyState()
	: StorageIndex(INDEX_NONE)
	, Storage(nullptr)
	{
	}

	const bool& Get() const;
	bool& Get();
	bool Set(bool InDirty);
	FTLLRN_RigTransformDirtyState& operator =(const FTLLRN_RigTransformDirtyState& InOther);

	int32 GetStorageIndex() const
	{
		return StorageIndex;
	}

private:

	void LinkStorage(const TArrayView<bool>& InStorage);
	void UnlinkStorage(FRigReusableElementStorage<bool>& InStorage);

	int32 StorageIndex;
	bool* Storage;

	friend struct FTLLRN_RigLocalAndGlobalDirtyState;
	friend class UTLLRN_RigHierarchy;
	friend class UTLLRN_RigHierarchyController;
	friend class FTLLRN_RigHierarchyPoseAdapter;
	friend struct FRigReusableElementStorage<bool>;
	friend class FTLLRN_ControlTLLRN_RigHierarchyRelinkElementStorage;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigLocalAndGlobalDirtyState
{
	GENERATED_BODY()

public:
	
	FTLLRN_RigLocalAndGlobalDirtyState()
	{
	}

	FTLLRN_RigLocalAndGlobalDirtyState& operator =(const FTLLRN_RigLocalAndGlobalDirtyState& InOther);

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "DirtyState")
	FTLLRN_RigTransformDirtyState Global;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "DirtyState")
	FTLLRN_RigTransformDirtyState Local;

private:

	void LinkStorage(const TArrayView<bool>& InStorage);
	void UnlinkStorage(FRigReusableElementStorage<bool>& InStorage);

	friend struct FTLLRN_RigCurrentAndInitialDirtyState;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigCurrentAndInitialDirtyState
{
	GENERATED_BODY()

public:
	
	FTLLRN_RigCurrentAndInitialDirtyState()
	{
	}

	bool& GetDirtyFlag(const ETLLRN_RigTransformType::Type InTransformType)
	{
		switch (InTransformType)
		{
			case ETLLRN_RigTransformType::CurrentLocal:
				return Current.Local.Get();

			case ETLLRN_RigTransformType::CurrentGlobal:
				return Current.Global.Get();

			case ETLLRN_RigTransformType::InitialLocal:
				return Initial.Local.Get();
				
			default:
				return Initial.Global.Get();
		}
	}

	const bool& GetDirtyFlag(const ETLLRN_RigTransformType::Type InTransformType) const
	{
		switch (InTransformType)
		{
			case ETLLRN_RigTransformType::CurrentLocal:
				return Current.Local.Get();

			case ETLLRN_RigTransformType::CurrentGlobal:
				return Current.Global.Get();

			case ETLLRN_RigTransformType::InitialLocal:
				return Initial.Local.Get();

			default:
				return Initial.Global.Get();
		}
	}

	bool IsDirty(const ETLLRN_RigTransformType::Type InTransformType) const
	{
		return GetDirtyFlag(InTransformType);
	}

	void MarkDirty(const ETLLRN_RigTransformType::Type InTransformType)
	{
		ensure(!(GetDirtyFlag(ETLLRN_RigTransformType::SwapLocalAndGlobal(InTransformType))));
		GetDirtyFlag(InTransformType) = true;
	}

	void MarkClean(const ETLLRN_RigTransformType::Type InTransformType)
	{
		GetDirtyFlag(InTransformType) = false;
	}

	FTLLRN_RigCurrentAndInitialDirtyState& operator =(const FTLLRN_RigCurrentAndInitialDirtyState& InOther);

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "DirtyState")
	FTLLRN_RigLocalAndGlobalDirtyState Current;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "DirtyState")
	FTLLRN_RigLocalAndGlobalDirtyState Initial;

private:

	void LinkStorage(const TArrayView<bool>& InStorage);
	void UnlinkStorage(FRigReusableElementStorage<bool>& InStorage);

	friend class UTLLRN_RigHierarchy;
	friend class UTLLRN_RigHierarchyController;
	friend struct FTLLRN_RigTransformElement;
	friend struct FTLLRN_RigControlElement;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigComputedTransform
{
	GENERATED_BODY()

public:
	
	FTLLRN_RigComputedTransform()
	: StorageIndex(INDEX_NONE)
	, Storage(nullptr)
	{}

	void Save(FArchive& Ar, const FTLLRN_RigTransformDirtyState& InDirtyState);
	void Load(FArchive& Ar, FTLLRN_RigTransformDirtyState& InDirtyState);
	
	const FTransform& Get() const;

	void Set(const FTransform& InTransform)
	{
#if WITH_EDITOR
		ensure(InTransform.GetRotation().IsNormalized());
#endif
		// ensure(!FMath::IsNearlyZero(InTransform.GetScale3D().X));
		// ensure(!FMath::IsNearlyZero(InTransform.GetScale3D().Y));
		// ensure(!FMath::IsNearlyZero(InTransform.GetScale3D().Z));
		if(Storage)
		{
			*Storage = InTransform;
		}
	}

	static bool Equals(const FTransform& A, const FTransform& B, const float InTolerance = 0.0001f)
	{
		return (A.GetTranslation() - B.GetTranslation()).IsNearlyZero(InTolerance) &&
			A.GetRotation().Equals(B.GetRotation(), InTolerance) &&
			(A.GetScale3D() - B.GetScale3D()).IsNearlyZero(InTolerance);
	}

	bool operator == (const FTLLRN_RigComputedTransform& Other) const
	{
		return Equals(Get(), Other.Get());
	}

	FTLLRN_RigComputedTransform& operator =(const FTLLRN_RigComputedTransform& InOther);

	int32 GetStorageIndex() const
	{
		return StorageIndex;
	}

private:

	void LinkStorage(const TArrayView<FTransform>& InStorage);
	void UnlinkStorage(FRigReusableElementStorage<FTransform>& InStorage);
	
	int32 StorageIndex;
	FTransform* Storage;
	
	friend struct FTLLRN_RigLocalAndGlobalTransform;
	friend class UTLLRN_RigHierarchy;
	friend class UTLLRN_RigHierarchyController;
	friend class FTLLRN_RigHierarchyPoseAdapter;
	friend struct FRigReusableElementStorage<FTransform>;
	friend class FTLLRN_ControlTLLRN_RigHierarchyRelinkElementStorage;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigLocalAndGlobalTransform
{
	GENERATED_BODY()

public:
	
	FTLLRN_RigLocalAndGlobalTransform()
    : Local()
    , Global()
	{}

	void Save(FArchive& Ar, const FTLLRN_RigLocalAndGlobalDirtyState& InDirtyState);
	void Load(FArchive& Ar, FTLLRN_RigLocalAndGlobalDirtyState& OutDirtyState);

	FTLLRN_RigLocalAndGlobalTransform& operator =(const FTLLRN_RigLocalAndGlobalTransform& InOther);

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Pose")
	FTLLRN_RigComputedTransform Local;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Pose")
	FTLLRN_RigComputedTransform Global;

private:
	
	void LinkStorage(const TArrayView<FTransform>& InStorage);
	void UnlinkStorage(FRigReusableElementStorage<FTransform>& InStorage);

	friend struct FTLLRN_RigCurrentAndInitialTransform;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigCurrentAndInitialTransform
{
	GENERATED_BODY()

public:
	
	FTLLRN_RigCurrentAndInitialTransform()
    : Current()
    , Initial()
	{}

	const FTLLRN_RigComputedTransform& operator[](const ETLLRN_RigTransformType::Type InTransformType) const
	{
		switch(InTransformType)
		{
			case ETLLRN_RigTransformType::CurrentLocal:
			{
				return Current.Local;
			}
			case ETLLRN_RigTransformType::CurrentGlobal:
			{
				return Current.Global;
			}
			case ETLLRN_RigTransformType::InitialLocal:
			{
				return Initial.Local;
			}
			default:
			{
				break;
			}
		}
		return Initial.Global;
	}

	FTLLRN_RigComputedTransform& operator[](const ETLLRN_RigTransformType::Type InTransformType)
	{
		switch(InTransformType)
		{
			case ETLLRN_RigTransformType::CurrentLocal:
			{
				return Current.Local;
			}
			case ETLLRN_RigTransformType::CurrentGlobal:
			{
				return Current.Global;
			}
			case ETLLRN_RigTransformType::InitialLocal:
			{
				return Initial.Local;
			}
			default:
			{
				break;
			}
		}
		return Initial.Global;
	}

	const FTransform& Get(const ETLLRN_RigTransformType::Type InTransformType) const
	{
		return operator[](InTransformType).Get();
	}

	void Set(const ETLLRN_RigTransformType::Type InTransformType, const FTransform& InTransform)
	{
		operator[](InTransformType).Set(InTransform);
	}

	void Save(FArchive& Ar, const FTLLRN_RigCurrentAndInitialDirtyState& InDirtyState);
	void Load(FArchive& Ar, FTLLRN_RigCurrentAndInitialDirtyState& OutDirtyState);

	bool operator == (const FTLLRN_RigCurrentAndInitialTransform& Other) const
	{
		return Current.Local  == Other.Current.Local
			&& Current.Global == Other.Current.Global
			&& Initial.Local  == Other.Initial.Local
			&& Initial.Global == Other.Initial.Global;
	}

	FTLLRN_RigCurrentAndInitialTransform& operator =(const FTLLRN_RigCurrentAndInitialTransform& InOther);

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Pose")
	FTLLRN_RigLocalAndGlobalTransform Current;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Pose")
	FTLLRN_RigLocalAndGlobalTransform Initial;
	
private:
	
	void LinkStorage(const TArrayView<FTransform>& InStorage);
	void UnlinkStorage(FRigReusableElementStorage<FTransform>& InStorage);

	friend class UTLLRN_RigHierarchy;
	friend class UTLLRN_RigHierarchyController;
	friend struct FTLLRN_RigTransformElement;
	friend struct FTLLRN_RigControlElement;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigPreferredEulerAngles
{
	GENERATED_BODY()

	static constexpr EEulerRotationOrder DefaultRotationOrder = EEulerRotationOrder::YZX;

	FTLLRN_RigPreferredEulerAngles()
	: RotationOrder(DefaultRotationOrder) // default for rotator
	, Current(FVector::ZeroVector)
	, Initial(FVector::ZeroVector)
	{}

	void Save(FArchive& Ar);
	void Load(FArchive& Ar);

	bool operator == (const FTLLRN_RigPreferredEulerAngles& Other) const
	{
		return RotationOrder == Other.RotationOrder &&
			Current == Other.Current &&
			Initial == Other.Initial;
	}

	void Reset();
	FVector& Get(bool bInitial = false) { return bInitial ? Initial : Current; }
	const FVector& Get(bool bInitial = false) const { return bInitial ? Initial : Current; }
	FRotator GetRotator(bool bInitial = false) const;
	FRotator SetRotator(const FRotator& InValue, bool bInitial = false, bool bFixEulerFlips = false);
	FVector GetAngles(bool bInitial = false, EEulerRotationOrder InRotationOrder = DefaultRotationOrder) const;
	void SetAngles(const FVector& InValue, bool bInitial = false, EEulerRotationOrder InRotationOrder = DefaultRotationOrder, bool bFixEulerFlips = false);
	void SetRotationOrder(EEulerRotationOrder InRotationOrder);


	FRotator GetRotatorFromQuat(const FQuat& InQuat) const;
	FQuat GetQuatFromRotator(const FRotator& InRotator) const;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Pose")
	EEulerRotationOrder RotationOrder;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Pose")
	FVector Current;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Pose")
	FVector Initial;
};


struct FTLLRN_RigBaseElement;
//typedef TArray<FTLLRN_RigBaseElement*> FTLLRN_RigBaseElementChildrenArray;
typedef TArray<FTLLRN_RigBaseElement*, TInlineAllocator<3>> FTLLRN_RigBaseElementChildrenArray;
//typedef TArray<FTLLRN_RigBaseElement*> FTLLRN_RigBaseElementParentArray;
typedef TArray<FTLLRN_RigBaseElement*, TInlineAllocator<1>> FTLLRN_RigBaseElementParentArray;

struct TLLRN_CONTROLRIG_API FTLLRN_RigElementHandle
{
public:

	FTLLRN_RigElementHandle()
		: Hierarchy(nullptr)
		, Key()
	{}

	FTLLRN_RigElementHandle(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigElementKey& InKey);
	FTLLRN_RigElementHandle(UTLLRN_RigHierarchy* InHierarchy, const FTLLRN_RigBaseElement* InElement);

	bool IsValid() const { return Get() != nullptr; }
	operator bool() const { return IsValid(); }
	
	const UTLLRN_RigHierarchy* GetHierarchy() const { return Hierarchy.Get(); }
	UTLLRN_RigHierarchy* GetHierarchy() { return Hierarchy.Get(); }
	const FTLLRN_RigElementKey& GetKey() const { return Key; }

	const FTLLRN_RigBaseElement* Get() const;
	FTLLRN_RigBaseElement* Get();

	template<typename T>
	T* Get()
	{
		return Cast<T>(Get());
	}

	template<typename T>
	const T* Get() const
	{
		return Cast<T>(Get());
	}

	template<typename T>
	T* GetChecked()
	{
		return CastChecked<T>(Get());
	}

	template<typename T>
	const T* GetChecked() const
	{
		return CastChecked<T>(Get());
	}

private:

	TWeakObjectPtr<UTLLRN_RigHierarchy> Hierarchy;
	FTLLRN_RigElementKey Key;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigBaseElement
{
	GENERATED_BODY()

public:

	enum EElementIndex
	{
		BaseElement,
		TransformElement,
		SingleParentElement,
		MultiParentElement,
		BoneElement,
		NullElement,
		ControlElement,
		CurveElement,
		PhysicsElement,
		ReferenceElement,
		ConnectorElement,
		SocketElement,

		Max
	};

	static const EElementIndex ElementTypeIndex;

	FTLLRN_RigBaseElement() = default;
	virtual ~FTLLRN_RigBaseElement();

	FTLLRN_RigBaseElement(const FTLLRN_RigBaseElement& InOther)
	{
		*this = InOther;
	}
	
	FTLLRN_RigBaseElement& operator=(const FTLLRN_RigBaseElement& InOther)
	{
		// We purposefully do not copy any non-UPROPERTY entries, including Owner. This is so that when the copied
		// element is deleted, the metadata is not deleted with it. These copies are purely intended for interfacing
		// with BP and details view wrappers. 
		// These copies are solely intended for UTLLRN_ControlRig::OnControlSelected_BP
		Key = InOther.Key;
		Index = InOther.Index;
		SubIndex = InOther.SubIndex;
		CreatedAtInstructionIndex = InOther.CreatedAtInstructionIndex;
		bSelected = InOther.bSelected;
		return *this;
	}
	
	virtual int32 GetElementTypeIndex() const { return ElementTypeIndex; }
	static int32 GetElementTypeCount() { return EElementIndex::Max; }

	enum ESerializationPhase
	{
		StaticData,
		InterElementData
	};

protected:
	// Only derived types should be able to construct this one.
	explicit FTLLRN_RigBaseElement(UTLLRN_RigHierarchy* InOwner, ETLLRN_RigElementType InElementType)
		: Owner(InOwner)
		, Key(InElementType)
	{
	}

	static bool IsClassOf(const FTLLRN_RigBaseElement* InElement)
	{
		return true;
	}

	// ReSharper disable once CppUE4ProbableMemoryIssuesWithUObject
	UTLLRN_RigHierarchy* Owner = nullptr;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = TLLRN_RigElement, meta = (AllowPrivateAccess = "true"))
	FTLLRN_RigElementKey Key;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = TLLRN_RigElement, meta = (AllowPrivateAccess = "true"))
	int32 Index = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = TLLRN_RigElement, meta = (AllowPrivateAccess = "true"))
	int32 SubIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Transient, Category = TLLRN_RigElement, meta = (AllowPrivateAccess = "true"))
	int32 CreatedAtInstructionIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Transient, Category = TLLRN_RigElement, meta = (AllowPrivateAccess = "true"))
	bool bSelected = false;

	// used for constructing / destructing the memory. typically == 1
	// Set by UTLLRN_RigHierarchy::NewElement.
	int32 OwnedInstances = 0;

	// Index into the child cache offset and count table in UTLLRN_RigHierarchy. Set by UTLLRN_RigHierarchy::UpdateCachedChildren
	int32 ChildCacheIndex = INDEX_NONE;

	// Index into the metadata storage for this element.
	int32 MetadataStorageIndex = INDEX_NONE;
	
	mutable FString CachedNameString;

public:

	UScriptStruct* GetElementStruct() const;
	void Serialize(FArchive& Ar, ESerializationPhase SerializationPhase);
	virtual void Save(FArchive& Ar, ESerializationPhase SerializationPhase);
	virtual void Load(FArchive& Ar, ESerializationPhase SerializationPhase);

	const FName& GetFName() const { return Key.Name; }
	const FString& GetName() const
	{
		if(CachedNameString.IsEmpty() && !Key.Name.IsNone())
		{
			CachedNameString = Key.Name.ToString();
		}
		return CachedNameString;
	}
	virtual const FName& GetDisplayName() const { return GetFName(); }
	ETLLRN_RigElementType GetType() const { return Key.Type; }
	const FTLLRN_RigElementKey& GetKey() const { return Key; }
	FTLLRN_RigElementKeyAndIndex GetKeyAndIndex() const { return {Key, Index}; };
	int32 GetIndex() const { return Index; }
	int32 GetSubIndex() const { return SubIndex; }
	bool IsSelected() const { return bSelected; }
	int32 GetCreatedAtInstructionIndex() const { return CreatedAtInstructionIndex; }
	bool IsProcedural() const { return CreatedAtInstructionIndex != INDEX_NONE; }
	const UTLLRN_RigHierarchy* GetOwner() const { return Owner; }
	UTLLRN_RigHierarchy* GetOwner() { return Owner; }

	// Metadata
	FTLLRN_RigBaseMetadata* GetMetadata(const FName& InName, ETLLRN_RigMetadataType InType = ETLLRN_RigMetadataType::Invalid);
	const FTLLRN_RigBaseMetadata* GetMetadata(const FName& InName, ETLLRN_RigMetadataType InType) const;
	bool SetMetadata(const FName& InName, ETLLRN_RigMetadataType InType, const void* InData, int32 InSize);
	FTLLRN_RigBaseMetadata* SetupValidMetadata(const FName& InName, ETLLRN_RigMetadataType InType);
	bool RemoveMetadata(const FName& InName);
	bool RemoveAllMetadata();
	
	void NotifyMetadataTagChanged(const FName& InTag, bool bAdded);

	virtual int32 GetNumTransforms() const { return 0; }
	virtual int32 GetNumCurves() const { return 0; }
	
	template<typename T>
	bool IsA() const { return T::IsClassOf(this); }

	bool IsTypeOf(ETLLRN_RigElementType InElementType) const
	{
		return Key.IsTypeOf(InElementType);
	}

	template<typename T>
    friend const T* Cast(const FTLLRN_RigBaseElement* InElement)
	{
		if(InElement)
		{
			if(InElement->IsA<T>())
			{
				return static_cast<const T*>(InElement);
			}
		}
		return nullptr;
	}

	template<typename T>
    friend T* Cast(FTLLRN_RigBaseElement* InElement)
	{
		if(InElement)
		{
			if(InElement->IsA<T>())
			{
				return static_cast<T*>(InElement);
			}
		}
		return nullptr;
	}

	template<typename T>
    friend const T* CastChecked(const FTLLRN_RigBaseElement* InElement)
	{
		const T* Element = Cast<T>(InElement);
		check(Element);
		return Element;
	}

	template<typename T>
    friend T* CastChecked(FTLLRN_RigBaseElement* InElement)
	{
		T* Element = Cast<T>(InElement);
		check(Element);
		return Element;
	}

	virtual void CopyPose(FTLLRN_RigBaseElement* InOther, bool bCurrent, bool bInitial, bool bWeights) {}

protected:
	// Used to initialize this base element during UTLLRN_RigHierarchy::CopyHierarchy. Once all elements are
	// initialized, the sub-class data copying is done using CopyFrom.
	void InitializeFrom(const FTLLRN_RigBaseElement* InOther);

	// helper function to be called as part of UTLLRN_RigHierarchy::CopyHierarchy
	virtual void CopyFrom(const FTLLRN_RigBaseElement* InOther);

	virtual void LinkStorage(const TArrayView<FTransform>& InTransforms, const TArrayView<bool>& InDirtyStates, const TArrayView<float>& InCurves) {}
	virtual void UnlinkStorage(FRigReusableElementStorage<FTransform>& InTransforms, FRigReusableElementStorage<bool>& InDirtyStates, FRigReusableElementStorage<float>& InCurves) {}

	friend class FTLLRN_ControlRigEditor;
	friend class UTLLRN_RigHierarchy;
	friend class UTLLRN_RigHierarchyController;
	friend struct FTLLRN_RigElementKeyAndIndex;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigTransformElement : public FTLLRN_RigBaseElement
{
	GENERATED_BODY()
	DECLARE_RIG_ELEMENT_METHODS(FTLLRN_RigTransformElement)

	static const EElementIndex ElementTypeIndex;

	FTLLRN_RigTransformElement() = default;
	FTLLRN_RigTransformElement(const FTLLRN_RigTransformElement& InOther)
	{
		*this = InOther;
	}
	FTLLRN_RigTransformElement& operator=(const FTLLRN_RigTransformElement& InOther)
	{
		Super::operator=(InOther);
		GetTransform() = InOther.GetTransform();
		GetDirtyState() = InOther.GetDirtyState();
		return *this;
	}
	virtual ~FTLLRN_RigTransformElement() override {}

	virtual void Save(FArchive& A, ESerializationPhase SerializationPhase) override;
	virtual void Load(FArchive& Ar, ESerializationPhase SerializationPhase) override;

	virtual void CopyPose(FTLLRN_RigBaseElement* InOther, bool bCurrent, bool bInitial, bool bWeights) override;

	// Current and Initial, both in Local and Global
	virtual int32 GetNumTransforms() const override { return 4; }

	const FTLLRN_RigCurrentAndInitialTransform& GetTransform() const;
	FTLLRN_RigCurrentAndInitialTransform& GetTransform();
	const FTLLRN_RigCurrentAndInitialDirtyState& GetDirtyState() const;
	FTLLRN_RigCurrentAndInitialDirtyState& GetDirtyState();

protected:
	
	FTLLRN_RigTransformElement(UTLLRN_RigHierarchy* InOwner, const ETLLRN_RigElementType InType) :
		FTLLRN_RigBaseElement(InOwner, InType)
	{}

	// Pose storage for this element.
	FTLLRN_RigCurrentAndInitialTransform PoseStorage;

	// Dirty state storage for this element.
	FTLLRN_RigCurrentAndInitialDirtyState PoseDirtyState;
	
	struct FElementToDirty
	{
		FElementToDirty()
			: Element(nullptr)
			, HierarchyDistance(INDEX_NONE)
		{}

		FElementToDirty(FTLLRN_RigTransformElement* InElement, int32 InHierarchyDistance = INDEX_NONE)
			: Element(InElement)
			, HierarchyDistance(InHierarchyDistance)
		{}

		bool operator ==(const FElementToDirty& Other) const
		{
			return Element == Other.Element;
		}

		bool operator !=(const FElementToDirty& Other) const
		{
			return Element != Other.Element;
		}
		
		FTLLRN_RigTransformElement* Element;
		int32 HierarchyDistance;
	};

	//typedef TArray<FElementToDirty> FElementsToDirtyArray;
	typedef TArray<FElementToDirty, TInlineAllocator<3>> FElementsToDirtyArray;  
	FElementsToDirtyArray ElementsToDirty;

	static bool IsClassOf(const FTLLRN_RigBaseElement* InElement)
	{
		return InElement->GetType() == ETLLRN_RigElementType::Bone ||
			InElement->GetType() == ETLLRN_RigElementType::Null ||
			InElement->GetType() == ETLLRN_RigElementType::Control ||
			InElement->GetType() == ETLLRN_RigElementType::Physics ||
			InElement->GetType() == ETLLRN_RigElementType::Reference ||
			InElement->GetType() == ETLLRN_RigElementType::Socket;
	}

	virtual void CopyFrom(const FTLLRN_RigBaseElement* InOther) override;

	virtual void LinkStorage(const TArrayView<FTransform>& InTransforms, const TArrayView<bool>& InDirtyStates, const TArrayView<float>& InCurves) override;
	virtual void UnlinkStorage(FRigReusableElementStorage<FTransform>& InTransforms, FRigReusableElementStorage<bool>& InDirtyStates, FRigReusableElementStorage<float>& InCurves) override;

	friend class UTLLRN_RigHierarchy;
	friend class UTLLRN_RigHierarchyController;
	friend struct FTLLRN_RigBaseElement;
	friend class FTLLRN_ControlRigEditor;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigSingleParentElement : public FTLLRN_RigTransformElement
{
	GENERATED_BODY()
	DECLARE_RIG_ELEMENT_METHODS(FTLLRN_RigSingleParentElement)

	static const EElementIndex ElementTypeIndex;

	FTLLRN_RigSingleParentElement() = default;
	virtual ~FTLLRN_RigSingleParentElement() override {}

	virtual void Save(FArchive& A, ESerializationPhase SerializationPhase) override;
	virtual void Load(FArchive& Ar, ESerializationPhase SerializationPhase) override;

	FTLLRN_RigTransformElement* ParentElement = nullptr;

protected:
	explicit FTLLRN_RigSingleParentElement(UTLLRN_RigHierarchy* InOwner, ETLLRN_RigElementType InType)
		: FTLLRN_RigTransformElement(InOwner, InType)
	{}

	virtual void CopyFrom(const FTLLRN_RigBaseElement* InOther) override;

	static bool IsClassOf(const FTLLRN_RigBaseElement* InElement)
	{
		return InElement->GetType() == ETLLRN_RigElementType::Bone ||
			InElement->GetType() == ETLLRN_RigElementType::Physics ||
			InElement->GetType() == ETLLRN_RigElementType::Reference ||
			InElement->GetType() == ETLLRN_RigElementType::Socket;
	}
	
	friend class UTLLRN_RigHierarchy;
	friend struct FTLLRN_RigBaseElement;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigElementWeight
{
public:
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Weight)
	float Location;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Weight)
	float Rotation;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Weight)
	float Scale;

	FTLLRN_RigElementWeight()
		: Location(1.f)
		, Rotation(1.f)
		, Scale(1.f)
	{}

	FTLLRN_RigElementWeight(float InWeight)
		: Location(InWeight)
		, Rotation(InWeight)
		, Scale(InWeight)
	{}

	FTLLRN_RigElementWeight(float InLocation, float InRotation, float InScale)
		: Location(InLocation)
		, Rotation(InRotation)
		, Scale(InScale)
	{}

	friend FArchive& operator <<(FArchive& Ar, FTLLRN_RigElementWeight& Weight)
	{
		Ar << Weight.Location;
		Ar << Weight.Rotation;
		Ar << Weight.Scale;
		return Ar;
	}

	bool AffectsLocation() const
	{
		return Location > SMALL_NUMBER;
	}

	bool AffectsRotation() const
	{
		return Rotation > SMALL_NUMBER;
	}

	bool AffectsScale() const
	{
		return Scale > SMALL_NUMBER;
	}

	bool IsAlmostZero() const
	{
		return !AffectsLocation() && !AffectsRotation() && !AffectsScale();
	}

	friend FTLLRN_RigElementWeight operator *(FTLLRN_RigElementWeight InWeight, float InScale)
	{
		return FTLLRN_RigElementWeight(InWeight.Location * InScale, InWeight.Rotation * InScale, InWeight.Scale * InScale);
	}

	friend FTLLRN_RigElementWeight operator *(float InScale, FTLLRN_RigElementWeight InWeight)
	{
		return FTLLRN_RigElementWeight(InWeight.Location * InScale, InWeight.Rotation * InScale, InWeight.Scale * InScale);
	}
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigElementParentConstraint
{
	GENERATED_BODY()

	FTLLRN_RigTransformElement* ParentElement;
	FTLLRN_RigElementWeight Weight;
	FTLLRN_RigElementWeight InitialWeight;
	mutable FTransform Cache;
	mutable bool bCacheIsDirty;
		
	FTLLRN_RigElementParentConstraint()
		: ParentElement(nullptr)
	{
		Cache = FTransform::Identity;
		bCacheIsDirty = true;
	}

	const FTLLRN_RigElementWeight& GetWeight(bool bInitial = false) const
	{
		return bInitial ? InitialWeight : Weight;
	}

	void CopyPose(const FTLLRN_RigElementParentConstraint& InOther, bool bCurrent, bool bInitial)
	{
		if(bCurrent)
		{
			Weight = InOther.Weight;
		}
		if(bInitial)
		{
			InitialWeight = InOther.InitialWeight;
		}
		bCacheIsDirty = true;
	}
};

#if URIGHIERARCHY_ENSURE_CACHE_VALIDITY
typedef TArray<FTLLRN_RigElementParentConstraint, TInlineAllocator<8>> FTLLRN_RigElementParentConstraintArray;
#else
typedef TArray<FTLLRN_RigElementParentConstraint, TInlineAllocator<1>> FTLLRN_RigElementParentConstraintArray;
#endif

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigMultiParentElement : public FTLLRN_RigTransformElement
{
	GENERATED_BODY()
	DECLARE_RIG_ELEMENT_METHODS(FTLLRN_RigMultiParentElement)

	static const EElementIndex ElementTypeIndex;

	FTLLRN_RigMultiParentElement() = default;	
	virtual ~FTLLRN_RigMultiParentElement() override {}

	virtual void Save(FArchive& A, ESerializationPhase SerializationPhase) override;
	virtual void Load(FArchive& Ar, ESerializationPhase SerializationPhase) override;

	virtual void CopyPose(FTLLRN_RigBaseElement* InOther, bool bCurrent, bool bInitial, bool bWeights) override;
	
	FTLLRN_RigElementParentConstraintArray ParentConstraints;
	TMap<FTLLRN_RigElementKey, int32> IndexLookup;

protected:
	explicit FTLLRN_RigMultiParentElement(UTLLRN_RigHierarchy* InOwner, const ETLLRN_RigElementType InType)
		: FTLLRN_RigTransformElement(InOwner, InType)
	{}

	virtual void CopyFrom(const FTLLRN_RigBaseElement* InOther) override;
	
private:
	static bool IsClassOf(const FTLLRN_RigBaseElement* InElement)
	{
		return InElement->GetType() == ETLLRN_RigElementType::Null ||
			InElement->GetType() == ETLLRN_RigElementType::Control;
	}

	friend class UTLLRN_RigHierarchy;
	friend struct FTLLRN_RigBaseElement;
};


USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigBoneElement : public FTLLRN_RigSingleParentElement
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_ELEMENT_METHODS(FTLLRN_RigBoneElement)

	static const EElementIndex ElementTypeIndex;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = TLLRN_RigElement)
	ETLLRN_RigBoneType BoneType = ETLLRN_RigBoneType::User;

	FTLLRN_RigBoneElement()
		: FTLLRN_RigBoneElement(nullptr)
	{}
	FTLLRN_RigBoneElement(const FTLLRN_RigBoneElement& InOther)
	{
		*this = InOther;
	}
	FTLLRN_RigBoneElement& operator=(const FTLLRN_RigBoneElement& InOther)
	{
		BoneType = InOther.BoneType;
		return *this;
	}
	
	virtual ~FTLLRN_RigBoneElement() override {}

	virtual void Save(FArchive& A, ESerializationPhase SerializationPhase) override;
	virtual void Load(FArchive& Ar, ESerializationPhase SerializationPhase) override;

private:
	explicit FTLLRN_RigBoneElement(UTLLRN_RigHierarchy* InOwner)
		: FTLLRN_RigSingleParentElement(InOwner, ETLLRN_RigElementType::Bone)
	{}

	virtual void CopyFrom(const FTLLRN_RigBaseElement* InOther) override;

	static bool IsClassOf(const FTLLRN_RigBaseElement* InElement)
	{
		return InElement->GetType() == ETLLRN_RigElementType::Bone;
	}

	friend class UTLLRN_RigHierarchy;
	friend struct FTLLRN_RigBaseElement;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigNullElement final : public FTLLRN_RigMultiParentElement
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_ELEMENT_METHODS(FTLLRN_RigNullElement)

	static const EElementIndex ElementTypeIndex;

	FTLLRN_RigNullElement() 
		: FTLLRN_RigNullElement(nullptr)
	{}

	virtual ~FTLLRN_RigNullElement() override {}

private:
	explicit FTLLRN_RigNullElement(UTLLRN_RigHierarchy* InOwner)
		: FTLLRN_RigMultiParentElement(InOwner, ETLLRN_RigElementType::Null)
	{}

	static bool IsClassOf(const FTLLRN_RigBaseElement* InElement)
	{
		return InElement->GetType() == ETLLRN_RigElementType::Null;
	}

	friend class UTLLRN_RigHierarchy;
	friend struct FTLLRN_RigBaseElement;
};

USTRUCT(BlueprintType)
struct FTLLRN_RigControlElementCustomization
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Customization)
	TArray<FTLLRN_RigElementKey> AvailableSpaces;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Customization)
	TArray<FTLLRN_RigElementKey> RemovedSpaces;
};

UENUM(BlueprintType)
enum class ETLLRN_RigControlTransformChannel : uint8
{
	TranslationX,
	TranslationY,
	TranslationZ,
	Pitch,
	Yaw,
	Roll,
	ScaleX,
	ScaleY,
	ScaleZ
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigControlSettings
{
	GENERATED_BODY()

	FTLLRN_RigControlSettings();

	void Save(FArchive& Ar);
	void Load(FArchive& Ar);

	friend uint32 GetTypeHash(const FTLLRN_RigControlSettings& Settings);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Control)
	ETLLRN_RigControlAnimationType AnimationType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Control, meta=(DisplayName="Value Type"))
	ETLLRN_RigControlType ControlType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Control)
	FName DisplayName;

	/** the primary axis to use for float controls */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Control)
	ETLLRN_RigControlAxis PrimaryAxis;

	/** If Created from a Curve  Container*/
	UPROPERTY(transient)
	bool bIsCurve;

	/** True if the control has limits. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Limits)
	TArray<FTLLRN_RigControlLimitEnabled> LimitEnabled;

	/**
	 * True if the limits should be drawn in debug.
	 * For this to be enabled you need to have at least one min and max limit turned on.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Limits)
	bool bDrawLimits;

	/** The minimum limit of the control's value */
	UPROPERTY(BlueprintReadWrite, Category = Limits)
	FTLLRN_RigControlValue MinimumValue;

	/** The maximum limit of the control's value */
	UPROPERTY(BlueprintReadWrite, Category = Limits)
	FTLLRN_RigControlValue MaximumValue;

	/** Set to true if the shape is currently visible in 3d */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Shape)
	bool bShapeVisible;

	/** Defines how the shape visibility should be changed */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Shape)
	ETLLRN_RigControlVisibility ShapeVisibility;

	/* This is optional UI setting - this doesn't mean this is always used, but it is optional for manipulation layer to use this*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Shape)
	FName ShapeName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Shape)
	FLinearColor ShapeColor;

	/** If the control is transient and only visible in the control rig editor */
	UPROPERTY(BlueprintReadWrite, Category = Control)
	bool bIsTransientControl;

	/** If the control is integer it can use this enum to choose values */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Control)
	TObjectPtr<UEnum> ControlEnum;

	/**
	 * The User interface customization used for a control
	 * This will be used as the default content for the space picker and other widgets
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Animation, meta = (DisplayName = "Customization"))
	FTLLRN_RigControlElementCustomization Customization;

	/**
	 * The list of driven controls for this proxy control.
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Animation)
	TArray<FTLLRN_RigElementKey> DrivenControls;

	/**
	 * The list of previously driven controls - prior to a procedural change
	 */
	TArray<FTLLRN_RigElementKey> PreviouslyDrivenControls;

	/**
	 * If set to true the animation channel will be grouped with the parent control in sequencer
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Animation)
	bool bGroupWithParentControl;

	/**
	 * Allow to space switch only to the available spaces
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Animation)
	bool bRestrictSpaceSwitching;

	/**
	 * Filtered Visible Transform channels. If this is empty everything is visible
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Animation)
	TArray<ETLLRN_RigControlTransformChannel> FilteredChannels;

	/**
	 * The euler rotation order this control prefers for animation, if we aren't using default UE rotator
	 */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Animation)
	EEulerRotationOrder PreferredRotationOrder;

	/**
	* Whether to use a specified rotation order or just use the default FRotator order and conversion functions
	*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Animation)
	bool bUsePreferredRotationOrder;

	/**
	* The euler rotation order this control prefers for animation if it is active. If not set then we use the default UE rotator.
	*/
	TOptional<EEulerRotationOrder> GetRotationOrder() const
	{
		TOptional<EEulerRotationOrder> RotationOrder;
		if (bUsePreferredRotationOrder)
		{
			RotationOrder = PreferredRotationOrder;
		}
		return RotationOrder;
	}

	/**
	*  Set the rotation order if the rotation is set otherwise use default rotator
	*/
	void SetRotationOrder(const TOptional<EEulerRotationOrder>& EulerRotation)
	{
		if (EulerRotation.IsSet())
		{
			bUsePreferredRotationOrder = true;
			PreferredRotationOrder = EulerRotation.GetValue();
		}
		else
		{
			bUsePreferredRotationOrder = false;
		}
	}
#if WITH_EDITORONLY_DATA
	/**
	 * Deprecated properties.
	 */
	UPROPERTY(meta=(DeprecatedProperty, DeprecationMessage = "Use animation_type instead."))
	bool bAnimatable_DEPRECATED = true;
	
	UPROPERTY(meta=(DeprecatedProperty, DeprecationMessage = "Use animation_type or shape_visible instead."))
	bool bShapeEnabled_DEPRECATED = true;
#endif
	
	/** Applies the limits expressed by these settings to a value */
	void ApplyLimits(FTLLRN_RigControlValue& InOutValue) const
	{
		InOutValue.ApplyLimits(LimitEnabled, ControlType, MinimumValue, MaximumValue);
	}

	/** Applies the limits expressed by these settings to a transform */
	void ApplyLimits(FTransform& InOutValue) const
	{
		FTLLRN_RigControlValue Value;
		Value.SetFromTransform(InOutValue, ControlType, PrimaryAxis);
		ApplyLimits(Value);
		InOutValue = Value.GetAsTransform(ControlType, PrimaryAxis);
	}

	FTLLRN_RigControlValue GetIdentityValue() const
	{
		FTLLRN_RigControlValue Value;
		Value.SetFromTransform(FTransform::Identity, ControlType, PrimaryAxis);
		return Value;
	}

	bool operator == (const FTLLRN_RigControlSettings& InOther) const;

	bool operator != (const FTLLRN_RigControlSettings& InOther) const
	{
		return !(*this == InOther);
	}

	void SetupLimitArrayForType(bool bLimitTranslation = false, bool bLimitRotation = false, bool bLimitScale = false);

	bool IsAnimatable() const
	{
		return (AnimationType == ETLLRN_RigControlAnimationType::AnimationControl) ||
			(AnimationType == ETLLRN_RigControlAnimationType::AnimationChannel);
	}

	bool ShouldBeGrouped() const
	{
		return IsAnimatable() && bGroupWithParentControl;
	}

	bool SupportsShape() const
	{
		return (AnimationType != ETLLRN_RigControlAnimationType::AnimationChannel) &&
			(ControlType != ETLLRN_RigControlType::Bool);
	}

	bool IsVisible() const
	{
		return SupportsShape() && bShapeVisible;
	}
	
	bool SetVisible(bool bVisible, bool bForce = false)
	{
		if(!bForce)
		{
			if(AnimationType == ETLLRN_RigControlAnimationType::ProxyControl)
			{
				if(ShapeVisibility == ETLLRN_RigControlVisibility::BasedOnSelection)
				{
					return false;
				}
			}
		}
		
		if(SupportsShape())
		{
			if(bShapeVisible == bVisible)
			{
				return false;
			}
			bShapeVisible = bVisible;
		}
		return SupportsShape();
	}

	bool IsSelectable(bool bRespectVisibility = true) const
	{
		return (AnimationType == ETLLRN_RigControlAnimationType::AnimationControl ||
			AnimationType == ETLLRN_RigControlAnimationType::ProxyControl) &&
			(IsVisible() || !bRespectVisibility);
	}

	void SetAnimationTypeFromDeprecatedData(bool bAnimatable, bool bShapeEnabled)
	{
		if(bAnimatable)
		{
			if(bShapeEnabled && (ControlType != ETLLRN_RigControlType::Bool))
			{
				AnimationType = ETLLRN_RigControlAnimationType::AnimationControl;
			}
			else
			{
				AnimationType = ETLLRN_RigControlAnimationType::AnimationChannel;
			}
		}
		else
		{
			AnimationType = ETLLRN_RigControlAnimationType::ProxyControl;
		}
	}
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigControlElement final : public FTLLRN_RigMultiParentElement
{
	public:
	
	GENERATED_BODY()
	DECLARE_RIG_ELEMENT_METHODS(FTLLRN_RigControlElement)

	static const EElementIndex ElementTypeIndex;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Control)
	FTLLRN_RigControlSettings Settings;

	// Current and Initial, both in Local and Global, for Pose, Offset and Shape
	virtual int32 GetNumTransforms() const override { return 12; }
	
	const FTLLRN_RigCurrentAndInitialTransform& GetOffsetTransform() const;
	FTLLRN_RigCurrentAndInitialTransform& GetOffsetTransform();
	const FTLLRN_RigCurrentAndInitialDirtyState& GetOffsetDirtyState() const;
	FTLLRN_RigCurrentAndInitialDirtyState& GetOffsetDirtyState();
	const FTLLRN_RigCurrentAndInitialTransform& GetShapeTransform() const;
	FTLLRN_RigCurrentAndInitialTransform& GetShapeTransform();
	const FTLLRN_RigCurrentAndInitialDirtyState& GetShapeDirtyState() const;
	FTLLRN_RigCurrentAndInitialDirtyState& GetShapeDirtyState();

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = TLLRN_RigElement)
	FTLLRN_RigPreferredEulerAngles PreferredEulerAngles;

	FTLLRN_RigControlElement()
		: FTLLRN_RigControlElement(nullptr)
	{ }
	
	FTLLRN_RigControlElement(const FTLLRN_RigControlElement& InOther)
	{
		*this = InOther;
	}
	
	FTLLRN_RigControlElement& operator=(const FTLLRN_RigControlElement& InOther)
	{
		Super::operator=(InOther);
		Settings = InOther.Settings;
		GetOffsetTransform() = InOther.GetOffsetTransform();
		GetShapeTransform() = InOther.GetShapeTransform();
		PreferredEulerAngles = InOther.PreferredEulerAngles;
		return *this;
	}

	virtual ~FTLLRN_RigControlElement() override {}
	
	virtual const FName& GetDisplayName() const override
	{
		if(!Settings.DisplayName.IsNone())
		{
			return Settings.DisplayName;
		}
		return FTLLRN_RigMultiParentElement::GetDisplayName();
	}

	bool IsAnimationChannel() const { return Settings.AnimationType == ETLLRN_RigControlAnimationType::AnimationChannel; }

	bool CanDriveControls() const { return Settings.AnimationType == ETLLRN_RigControlAnimationType::ProxyControl || Settings.AnimationType == ETLLRN_RigControlAnimationType::AnimationControl; }

	bool CanTreatAsAdditive() const
	{
		if (Settings.ControlType == ETLLRN_RigControlType::Bool)
		{
			return false;
		}
		if (Settings.ControlType == ETLLRN_RigControlType::Integer && Settings.ControlEnum != nullptr)
		{
			return false;
		}
		return true;
	}

	virtual void Save(FArchive& A, ESerializationPhase SerializationPhase) override;
	virtual void Load(FArchive& Ar, ESerializationPhase SerializationPhase) override;

	virtual void CopyPose(FTLLRN_RigBaseElement* InOther, bool bCurrent, bool bInitial, bool bWeights) override;

protected:

	virtual void LinkStorage(const TArrayView<FTransform>& InTransforms, const TArrayView<bool>& InDirtyStates, const TArrayView<float>& InCurves) override;
	virtual void UnlinkStorage(FRigReusableElementStorage<FTransform>& InTransforms, FRigReusableElementStorage<bool>& InDirtyStates, FRigReusableElementStorage<float>& InCurves) override;
	
private:
	explicit FTLLRN_RigControlElement(UTLLRN_RigHierarchy* InOwner)
		: FTLLRN_RigMultiParentElement(InOwner, ETLLRN_RigElementType::Control)
	{ }

	virtual void CopyFrom(const FTLLRN_RigBaseElement* InOther) override;

	static bool IsClassOf(const FTLLRN_RigBaseElement* InElement)
	{
		return InElement->GetType() == ETLLRN_RigElementType::Control;
	}

	// Offset storage for this element.
	FTLLRN_RigCurrentAndInitialTransform OffsetStorage;
	FTLLRN_RigCurrentAndInitialDirtyState OffsetDirtyState;

	// Shape storage for this element.
	FTLLRN_RigCurrentAndInitialTransform ShapeStorage;
	FTLLRN_RigCurrentAndInitialDirtyState ShapeDirtyState;

	friend class UTLLRN_RigHierarchy;
	friend class UTLLRN_RigHierarchyController;
	friend struct FTLLRN_RigBaseElement;
	friend class FTLLRN_ControlRigEditor;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigCurveElement final : public FTLLRN_RigBaseElement
{
	GENERATED_BODY()
	DECLARE_RIG_ELEMENT_METHODS(FTLLRN_RigCurveElement)

	static const EElementIndex ElementTypeIndex;

	FTLLRN_RigCurveElement()
		: FTLLRN_RigCurveElement(nullptr)
	{}
	
	virtual ~FTLLRN_RigCurveElement() override {}

	virtual int32 GetNumCurves() const override { return 1; }

	virtual void Save(FArchive& A, ESerializationPhase SerializationPhase) override;
	virtual void Load(FArchive& Ar, ESerializationPhase SerializationPhase) override;

	virtual void CopyPose(FTLLRN_RigBaseElement* InOther, bool bCurrent, bool bInitial, bool bWeights) override;

	const float& Get() const;
	void Set(const float& InValue, bool InValueIsSet = true);

	bool IsValueSet() const { return bIsValueSet; }

	int32 GetStorageIndex() const
	{
		return StorageIndex;
	}

protected:

	virtual void LinkStorage(const TArrayView<FTransform>& InTransforms, const TArrayView<bool>& InDirtyStates, const TArrayView<float>& InCurves) override;
	virtual void UnlinkStorage(FRigReusableElementStorage<FTransform>& InTransforms, FRigReusableElementStorage<bool>& InDirtyStates, FRigReusableElementStorage<float>& InCurves) override;
	
private:
	FTLLRN_RigCurveElement(UTLLRN_RigHierarchy* InOwner)
		: FTLLRN_RigBaseElement(InOwner, ETLLRN_RigElementType::Curve)
	{}
	
	virtual void CopyFrom(const FTLLRN_RigBaseElement* InOther) override;

	static bool IsClassOf(const FTLLRN_RigBaseElement* InElement)
	{
		return InElement->GetType() == ETLLRN_RigElementType::Curve;
	}

	// Set to true if the value was actually set. Used to carry back and forth blend curve
	// value validity state.
	bool bIsValueSet = true;

	int32 StorageIndex = INDEX_NONE;
	float* Storage = nullptr;

	friend class UTLLRN_RigHierarchy;
	friend class UTLLRN_RigHierarchyController;
	friend struct FTLLRN_RigBaseElement;
	friend class FTLLRN_RigHierarchyPoseAdapter;
	friend struct FRigReusableElementStorage<float>;
	friend class FTLLRN_ControlTLLRN_RigHierarchyRelinkElementStorage;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigPhysicsSolverID
{
public:
	
	GENERATED_BODY()
	
	FTLLRN_RigPhysicsSolverID()
		: Guid()
	{
	}

	explicit FTLLRN_RigPhysicsSolverID(const FGuid& InGuid)
		: Guid(InGuid)
	{
	}

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Physics, meta=(ShowOnlyInnerProperties))
	FGuid Guid;

	bool IsValid() const
	{
		return Guid.IsValid();
	}

	FString ToString() const
	{
		return Guid.ToString();
	}

	bool operator == (const FTLLRN_RigPhysicsSolverID& InOther) const
	{
		return Guid == InOther.Guid;
	}

	bool operator != (const FTLLRN_RigPhysicsSolverID& InOther) const
	{
		return !(*this == InOther);
	}

	friend FArchive& operator <<(FArchive& Ar, FTLLRN_RigPhysicsSolverID& ID)
	{
		Ar << ID.Guid;
		return Ar;
	}

	friend uint32 GetTypeHash(const FTLLRN_RigPhysicsSolverID& InID)
	{
		return GetTypeHash(InID.Guid);
	}
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigPhysicsSolverDescription
{
public:
	
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Physics, meta=(ShowOnlyInnerProperties))
	FTLLRN_RigPhysicsSolverID ID;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Physics, meta=(ShowOnlyInnerProperties))
	FName Name;

	FTLLRN_RigPhysicsSolverDescription()
	: ID()
	, Name(NAME_None)
	{}
	
	FTLLRN_RigPhysicsSolverDescription(const FTLLRN_RigPhysicsSolverDescription& InOther)
	{
		*this = InOther;
	}
	
	FTLLRN_RigPhysicsSolverDescription& operator=(const FTLLRN_RigPhysicsSolverDescription& InOther)
	{
		CopyFrom(&InOther);
		return *this;
	}

	void Serialize(FArchive& Ar);
	void Save(FArchive& Ar);
	void Load(FArchive& Ar);
	friend FArchive& operator<<(FArchive& Ar, FTLLRN_RigPhysicsSolverDescription& P)
	{
		P.Serialize(Ar);
		return Ar;
	}

	static FGuid MakeGuid(const FString& InObjectPath, const FName& InSolverName);
	static FTLLRN_RigPhysicsSolverID MakeID(const FString& InObjectPath, const FName& InSolverName);

private:

	void CopyFrom(const FTLLRN_RigPhysicsSolverDescription* InOther);
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigPhysicsSettings
{
	GENERATED_BODY()

	FTLLRN_RigPhysicsSettings();

	void Save(FArchive& Ar);
	void Load(FArchive& Ar);


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Control)
	float Mass;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigPhysicsElement : public FTLLRN_RigSingleParentElement
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_ELEMENT_METHODS(FTLLRN_RigPhysicsElement)

	static const EElementIndex ElementTypeIndex;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Physics, meta=(ShowOnlyInnerProperties))
	FTLLRN_RigPhysicsSolverID Solver;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Physics, meta=(ShowOnlyInnerProperties))
	FTLLRN_RigPhysicsSettings Settings;
	
	FTLLRN_RigPhysicsElement()
        : FTLLRN_RigPhysicsElement(nullptr)
	{ }
	
	FTLLRN_RigPhysicsElement(const FTLLRN_RigPhysicsElement& InOther)
	{
		*this = InOther;
	}
	
	FTLLRN_RigPhysicsElement& operator=(const FTLLRN_RigPhysicsElement& InOther)
	{
		Super::operator=(InOther);
		Solver = InOther.Solver;
		Settings = InOther.Settings;
		return *this;
	}
	
	virtual ~FTLLRN_RigPhysicsElement() override {}

	virtual void Save(FArchive& A, ESerializationPhase SerializationPhase) override;
	virtual void Load(FArchive& Ar, ESerializationPhase SerializationPhase) override;

private:
	explicit FTLLRN_RigPhysicsElement(UTLLRN_RigHierarchy* InOwner)
		: FTLLRN_RigSingleParentElement(InOwner, ETLLRN_RigElementType::Physics)
	{ }
	
	virtual void CopyFrom(const FTLLRN_RigBaseElement* InOther) override;

	static bool IsClassOf(const FTLLRN_RigBaseElement* InElement)
	{
		return InElement->GetType() == ETLLRN_RigElementType::Physics;
	}

	friend class UTLLRN_RigHierarchy;
	friend struct FTLLRN_RigBaseElement;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigReferenceElement final : public FTLLRN_RigSingleParentElement
{
public:
	
	GENERATED_BODY()
	DECLARE_RIG_ELEMENT_METHODS(FTLLRN_RigReferenceElement)

	static const EElementIndex ElementTypeIndex;

    FTLLRN_RigReferenceElement()
        : FTLLRN_RigReferenceElement(nullptr)
	{ }
	
	virtual ~FTLLRN_RigReferenceElement() override {}

	virtual void Save(FArchive& A, ESerializationPhase SerializationPhase) override;
	virtual void Load(FArchive& Ar, ESerializationPhase SerializationPhase) override;

	FTransform GetReferenceWorldTransform(const FRigVMExecuteContext* InContext, bool bInitial) const;

	virtual void CopyPose(FTLLRN_RigBaseElement* InOther, bool bCurrent, bool bInitial, bool bWeights) override;

private:
	explicit FTLLRN_RigReferenceElement(UTLLRN_RigHierarchy* InOwner)
		: FTLLRN_RigSingleParentElement(InOwner, ETLLRN_RigElementType::Reference)
	{ }

	FRigReferenceGetWorldTransformDelegate GetWorldTransformDelegate;

	virtual void CopyFrom(const FTLLRN_RigBaseElement* InOther) override;

	static bool IsClassOf(const FTLLRN_RigBaseElement* InElement)
	{
		return InElement->GetType() == ETLLRN_RigElementType::Reference;
	}

	friend class UTLLRN_RigHierarchy;
	friend class UTLLRN_RigHierarchyController;
	friend struct FTLLRN_RigBaseElement;
};

UENUM(BlueprintType)
enum class ETLLRN_ConnectorType : uint8
{
	Primary, // Single primary connector, non-optional and always visible. When dropped on another element, this connector will resolve to that element.
	Secondary, // Could be multiple, can auto-solve (visible if not solved), can be optional
};


USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigConnectorSettings
{
	GENERATED_BODY()

	FTLLRN_RigConnectorSettings();
	static FTLLRN_RigConnectorSettings DefaultSettings();

	void Save(FArchive& Ar);
	void Load(FArchive& Ar);

	friend uint32 GetTypeHash(const FTLLRN_RigConnectorSettings& Settings);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	FString Description;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	ETLLRN_ConnectorType Type;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	bool bOptional;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	TArray<FTLLRN_RigConnectionRuleStash> Rules;

	bool operator == (const FTLLRN_RigConnectorSettings& InOther) const;

	bool operator != (const FTLLRN_RigConnectorSettings& InOther) const
	{
		return !(*this == InOther);
	}

	template<typename T>
	int32 AddRule(const T& InRule)
	{
		return Rules.Emplace(&InRule);
	}

	uint32 GetRulesHash() const;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigConnectorState
{
	GENERATED_BODY()

	FTLLRN_RigConnectorState()
		: Name(NAME_None)
	{}
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	FName Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	FTLLRN_RigElementKey ResolvedTarget;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	FTLLRN_RigConnectorSettings Settings;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigConnectorElement final : public FTLLRN_RigBaseElement
{
	GENERATED_BODY()
	DECLARE_RIG_ELEMENT_METHODS(FTLLRN_RigConnectorElement)

	static const EElementIndex ElementTypeIndex;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = Control)
	FTLLRN_RigConnectorSettings Settings;
	
	FTLLRN_RigConnectorElement()
		: FTLLRN_RigConnectorElement(nullptr)
	{}
	FTLLRN_RigConnectorElement(const FTLLRN_RigConnectorElement& InOther)
	{
		*this = InOther;
	}
	FTLLRN_RigConnectorElement& operator=(const FTLLRN_RigConnectorElement& InOther)
	{
		Super::operator=(InOther);
		Settings = InOther.Settings;
		return *this;
	}

	virtual ~FTLLRN_RigConnectorElement() override {}
	
	virtual void Save(FArchive& A, ESerializationPhase SerializationPhase) override;
	virtual void Load(FArchive& Ar, ESerializationPhase SerializationPhase) override;

	FTLLRN_RigConnectorState GetConnectorState(const UTLLRN_RigHierarchy* InHierarchy) const;

	bool IsPrimary() const { return Settings.Type == ETLLRN_ConnectorType::Primary; }
	bool IsSecondary() const { return Settings.Type == ETLLRN_ConnectorType::Secondary; }
	bool IsOptional() const { return IsSecondary() && Settings.bOptional; }

private:
	explicit FTLLRN_RigConnectorElement(UTLLRN_RigHierarchy* InOwner)
		: FTLLRN_RigBaseElement(InOwner, ETLLRN_RigElementType::Connector)
	{ }
	
	virtual void CopyFrom(const FTLLRN_RigBaseElement* InOther) override;

	static bool IsClassOf(const FTLLRN_RigBaseElement* InElement)
	{
		return InElement->GetType() == ETLLRN_RigElementType::Connector;
	}

	friend class UTLLRN_RigHierarchy;
	friend struct FTLLRN_RigBaseElement;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigSocketState
{
	GENERATED_BODY()
	
	FTLLRN_RigSocketState();
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	FName Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	FTLLRN_RigElementKey Parent;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	FTransform InitialLocalTransform;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	FLinearColor Color;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Connector)
	FString Description;
};

USTRUCT(BlueprintType)
struct TLLRN_CONTROLRIG_API FTLLRN_RigSocketElement final : public FTLLRN_RigSingleParentElement
{
	GENERATED_BODY()
	DECLARE_RIG_ELEMENT_METHODS(FTLLRN_RigSocketElement)

	static const EElementIndex ElementTypeIndex;
	static const FName ColorMetaName;
	static const FName DescriptionMetaName;
	static const FName DesiredParentMetaName;
	static const FLinearColor SocketDefaultColor;

	FTLLRN_RigSocketElement()
		: FTLLRN_RigSocketElement(nullptr)
	{}
			
	virtual ~FTLLRN_RigSocketElement() override {}
	
	virtual void Save(FArchive& A, ESerializationPhase SerializationPhase) override;
	virtual void Load(FArchive& Ar, ESerializationPhase SerializationPhase) override;

	FTLLRN_RigSocketState GetSocketState(const UTLLRN_RigHierarchy* InHierarchy) const;

	FLinearColor GetColor(const UTLLRN_RigHierarchy* InHierarchy) const;
	void SetColor(const FLinearColor& InColor, UTLLRN_RigHierarchy* InHierarchy, bool bNotify = true);

	FString GetDescription(const UTLLRN_RigHierarchy* InHierarchy) const;
	void SetDescription(const FString& InDescription, UTLLRN_RigHierarchy* InHierarchy, bool bNotify = true);

private:
	explicit FTLLRN_RigSocketElement(UTLLRN_RigHierarchy* InOwner)
		: FTLLRN_RigSingleParentElement(InOwner, ETLLRN_RigElementType::Socket)
	{ }
	
	virtual void CopyFrom(const FTLLRN_RigBaseElement* InOther) override;

	static bool IsClassOf(const FTLLRN_RigBaseElement* InElement)
	{
		return InElement->GetType() == ETLLRN_RigElementType::Socket;
	}

	friend class UTLLRN_RigHierarchy;
	friend struct FTLLRN_RigBaseElement;
};


USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigHierarchyCopyPasteContentPerElement
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTLLRN_RigElementKey Key;

	UPROPERTY()
	FString Content;

	UPROPERTY()
	TArray<FTLLRN_RigElementKey> Parents;

	UPROPERTY()
	TArray<FTLLRN_RigElementWeight> ParentWeights;

	UPROPERTY()
	TArray<FTransform> Poses;

	UPROPERTY()
	TArray<bool> DirtyStates;
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_RigHierarchyCopyPasteContent
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FTLLRN_RigHierarchyCopyPasteContentPerElement> Elements;

	// Maintain properties below for backwards compatibility pre-5.0
	UPROPERTY()
	TArray<ETLLRN_RigElementType> Types;

	UPROPERTY()
	TArray<FString> Contents;

	UPROPERTY()
	TArray<FTransform> LocalTransforms;

	UPROPERTY()
	TArray<FTransform> GlobalTransforms;
};
