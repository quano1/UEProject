// Copyright Epic Games, Inc. All Rights Reserved.

#include "Rigs/TLLRN_RigHierarchyPoseAdapter.h"
#include "Rigs/TLLRN_RigHierarchy.h"

UTLLRN_RigHierarchy* FTLLRN_RigHierarchyPoseAdapter::GetHierarchy() const
{
	if(WeakHierarchy.IsValid())
	{
		return WeakHierarchy.Get();
	}
	return nullptr;
}

void FTLLRN_RigHierarchyPoseAdapter::PostLinked(UTLLRN_RigHierarchy* InHierarchy)
{
	WeakHierarchy = InHierarchy; 
}

void FTLLRN_RigHierarchyPoseAdapter::PreUnlinked(UTLLRN_RigHierarchy* InHierarchy)
{
}

void FTLLRN_RigHierarchyPoseAdapter::PostUnlinked(UTLLRN_RigHierarchy* InHierarchy)
{
	WeakHierarchy.Reset();
}

TTuple<FTLLRN_RigComputedTransform*, FTLLRN_RigTransformDirtyState*> FTLLRN_RigHierarchyPoseAdapter::GetElementTransformStorage(
	const FTLLRN_RigElementKeyAndIndex& InKeyAndIndex, ETLLRN_RigTransformType::Type InTransformType, ETLLRN_RigTransformStorageType::Type InStorageType) const
{
	if(UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		return Hierarchy->GetElementTransformStorage(InKeyAndIndex, InTransformType, InStorageType);
	}
	return {nullptr, nullptr};
}

bool FTLLRN_RigHierarchyPoseAdapter::RelinkTransformStorage(const FTLLRN_RigElementKeyAndIndex& InKeyAndIndex, ETLLRN_RigTransformType::Type InTransformType,
                                                      ETLLRN_RigTransformStorageType::Type InStorageType, FTransform* InTransformStorage, bool* InDirtyFlagStorage)
{
	TArray<TTuple<FTLLRN_RigElementKeyAndIndex, ETLLRN_RigTransformType::Type, ETLLRN_RigTransformStorageType::Type, FTransform*, bool*>> Data =
		{{InKeyAndIndex, InTransformType, InStorageType, InTransformStorage, InDirtyFlagStorage}};
	return RelinkTransformStorage(Data);
}

bool FTLLRN_RigHierarchyPoseAdapter::RestoreTransformStorage(const FTLLRN_RigElementKeyAndIndex& InKeyAndIndex, ETLLRN_RigTransformType::Type InTransformType,
	ETLLRN_RigTransformStorageType::Type InStorageType, bool bUpdateElementStorage)
{
	TArray<TTuple<FTLLRN_RigElementKeyAndIndex, ETLLRN_RigTransformType::Type, ETLLRN_RigTransformStorageType::Type>> Data =
		{{InKeyAndIndex, InTransformType, InStorageType}};
	return RestoreTransformStorage(Data, bUpdateElementStorage);
}

bool FTLLRN_RigHierarchyPoseAdapter::RelinkTransformStorage(
	const TArrayView<TTuple<FTLLRN_RigElementKeyAndIndex, ETLLRN_RigTransformType::Type, ETLLRN_RigTransformStorageType::Type, FTransform*, bool*>>& InData)
{
	if(UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		TArray<int32> TransformIndicesToDeallocate;
		TArray<int32> DirtyStateIndicesToDeallocate;
		TransformIndicesToDeallocate.Reserve(InData.Num());
		DirtyStateIndicesToDeallocate.Reserve(InData.Num());
		
		bool bPerformedChange = false;
		for(const TTuple<FTLLRN_RigElementKeyAndIndex, ETLLRN_RigTransformType::Type, ETLLRN_RigTransformStorageType::Type, FTransform*, bool*>& Tuple : InData)
		{
			auto CurrentStorage = Hierarchy->GetElementTransformStorage(Tuple.Get<0>(), Tuple.Get<1>(), Tuple.Get<2>());

			if(FTransform* NewTransformStorage = Tuple.Get<3>())
			{
				const FTransform PreviousTransform = CurrentStorage.Get<0>()->Get();
				if(Hierarchy->ElementTransforms.Contains(CurrentStorage.Get<0>()))
				{
					TransformIndicesToDeallocate.Add(CurrentStorage.Get<0>()->GetStorageIndex());
				}
				CurrentStorage.Get<0>()->StorageIndex = INDEX_NONE;
				CurrentStorage.Get<0>()->Storage = NewTransformStorage;
				CurrentStorage.Get<0>()->Set(PreviousTransform);
				bPerformedChange = true;
			}
			if(bool* NewDirtyStateStorage = Tuple.Get<4>())
			{
				const bool bPreviousState = CurrentStorage.Get<1>()->Get();
				if(Hierarchy->ElementDirtyStates.Contains(CurrentStorage.Get<1>()))
				{
					DirtyStateIndicesToDeallocate.Add(CurrentStorage.Get<1>()->GetStorageIndex());
				}
				CurrentStorage.Get<1>()->StorageIndex = INDEX_NONE;
				CurrentStorage.Get<1>()->Storage = NewDirtyStateStorage;
				CurrentStorage.Get<1>()->Set(bPreviousState);
				bPerformedChange = true;
			}
		}

		Hierarchy->ElementTransforms.Deallocate(TransformIndicesToDeallocate);
		Hierarchy->ElementDirtyStates.Deallocate(DirtyStateIndicesToDeallocate);
		return bPerformedChange;
	}
	return false;
}

bool FTLLRN_RigHierarchyPoseAdapter::RestoreTransformStorage(
	const TArrayView<TTuple<FTLLRN_RigElementKeyAndIndex, ETLLRN_RigTransformType::Type, ETLLRN_RigTransformStorageType::Type>>& InData, bool bUpdateElementStorage)
{
	if(UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		TArray<TTuple<FTLLRN_RigComputedTransform*, FTLLRN_RigTransformDirtyState*>> StoragePerElement;
		StoragePerElement.Reserve(InData.Num());
		for(const TTuple<FTLLRN_RigElementKeyAndIndex, ETLLRN_RigTransformType::Type, ETLLRN_RigTransformStorageType::Type>& Tuple : InData)
		{
			const TTuple<FTLLRN_RigComputedTransform*, FTLLRN_RigTransformDirtyState*> CurrentStorage =
				Hierarchy->GetElementTransformStorage(Tuple.Get<0>(), Tuple.Get<1>(), Tuple.Get<2>());

			if(CurrentStorage.Get<0>() == nullptr || CurrentStorage.Get<1>() == nullptr)
			{
				continue;
			}
			if(Hierarchy->ElementTransforms.Contains(CurrentStorage.Get<0>()) ||
				Hierarchy->ElementDirtyStates.Contains(CurrentStorage.Get<1>()))
			{
				continue;
			}
			StoragePerElement.Add(CurrentStorage);
		}

		if(StoragePerElement.IsEmpty())
		{
			return false;
		}

		const TArray<int32, TInlineAllocator<4>> NewTransformIndices = Hierarchy->ElementTransforms.Allocate(StoragePerElement.Num(), FTransform::Identity);
		const TArray<int32, TInlineAllocator<4>> NewDirtyStateIndices = Hierarchy->ElementDirtyStates.Allocate(StoragePerElement.Num(), false);
		check(StoragePerElement.Num() == NewTransformIndices.Num());
		check(StoragePerElement.Num() == NewDirtyStateIndices.Num());
		for(int32 Index = 0; Index < StoragePerElement.Num(); Index++)
		{
			FTLLRN_RigComputedTransform* ComputedTransform = StoragePerElement[Index].Get<0>();
			FTLLRN_RigTransformDirtyState* DirtyState = StoragePerElement[Index].Get<1>();

			const FTransform PreviousTransform = ComputedTransform->Get();
			const bool bPreviousState = DirtyState->Get();

			ComputedTransform->StorageIndex = NewTransformIndices[Index];
			ComputedTransform->Storage = &Hierarchy->ElementTransforms[ComputedTransform->StorageIndex];

			DirtyState->StorageIndex = NewDirtyStateIndices[Index];
			DirtyState->Storage = &Hierarchy->ElementDirtyStates[DirtyState->StorageIndex];
			
			ComputedTransform->Set(PreviousTransform);
			DirtyState->Set(bPreviousState);
		}
		
		if(bUpdateElementStorage)
		{
			(void)UpdateHierarchyStorage();
			(void)SortHierarchyStorage();
		}
		return true;
	}
	return false;
}

bool FTLLRN_RigHierarchyPoseAdapter::RelinkCurveStorage(const FTLLRN_RigElementKeyAndIndex& InKeyAndIndex, float* InCurveStorage)
{
	TArray<TTuple<FTLLRN_RigElementKeyAndIndex, float*>> Data = {{InKeyAndIndex, InCurveStorage}};
	return RelinkCurveStorage(Data);
}

bool FTLLRN_RigHierarchyPoseAdapter::RestoreCurveStorage(const FTLLRN_RigElementKeyAndIndex& InKeyAndIndex, bool bUpdateElementStorage)
{
	TArray<FTLLRN_RigElementKeyAndIndex> Data = {InKeyAndIndex};
	return RestoreCurveStorage(Data, bUpdateElementStorage);
}

bool FTLLRN_RigHierarchyPoseAdapter::RelinkCurveStorage(const TArrayView<TTuple<FTLLRN_RigElementKeyAndIndex, float*>>& InData)
{
	if(UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		TArray<int32> CurveIndicesToDeallocate;
		CurveIndicesToDeallocate.Reserve(InData.Num());
		
		bool bPerformedChange = false;
		for(const TTuple<FTLLRN_RigElementKeyAndIndex, float*>& Tuple : InData)
		{
			FTLLRN_RigCurveElement* CurveElement = Hierarchy->Get<FTLLRN_RigCurveElement>(Tuple.Get<0>());
			const float PreviousValue = CurveElement->Get();

			if(float* NewCurveStorage = Tuple.Get<1>())
			{
				if(Hierarchy->ElementCurves.Contains(CurveElement))
				{
					CurveIndicesToDeallocate.Add(CurveElement->GetStorageIndex());
				}
				CurveElement->StorageIndex = INDEX_NONE;
				CurveElement->Storage = NewCurveStorage;
				bPerformedChange = true;
			}

			CurveElement->Set(PreviousValue, CurveElement->bIsValueSet);
		}

		Hierarchy->ElementCurves.Deallocate(CurveIndicesToDeallocate);
		return bPerformedChange;
	}
	return false;
}

bool FTLLRN_RigHierarchyPoseAdapter::RestoreCurveStorage(const TArrayView<FTLLRN_RigElementKeyAndIndex>& InData, bool bUpdateElementStorage)
{
	if(UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		TArray<FTLLRN_RigCurveElement*> Curves;
		Curves.Reserve(InData.Num());
		for(const FTLLRN_RigElementKeyAndIndex& KeyAndIndex : InData)
		{
			if(FTLLRN_RigCurveElement* CurveElement = Hierarchy->Get<FTLLRN_RigCurveElement>(KeyAndIndex))
			{
				if(!Hierarchy->ElementCurves.Contains(CurveElement))
				{
					Curves.Add(CurveElement);
				}
			}
		}

		if(Curves.IsEmpty())
		{
			return false;
		}

		const TArray<int32, TInlineAllocator<4>> NewCurveIndices = Hierarchy->ElementCurves.Allocate(Curves.Num(), 0.f);
		check(Curves.Num() == NewCurveIndices.Num());
		for(int32 Index = 0; Index < Curves.Num(); Index++)
		{
			FTLLRN_RigCurveElement* CurveElement = Curves[Index];
			const float PreviousValue = CurveElement->Get();
			CurveElement->StorageIndex = NewCurveIndices[Index];
			CurveElement->Storage = &Hierarchy->ElementCurves[CurveElement->StorageIndex];
			CurveElement->Set(PreviousValue, CurveElement->bIsValueSet);
		}

		if(bUpdateElementStorage)
		{
			(void)UpdateHierarchyStorage();
		}
		return true;
	}
	return false;
}

bool FTLLRN_RigHierarchyPoseAdapter::SortHierarchyStorage()
{
	if(UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		return Hierarchy->SortElementStorage();
	}
	return false;
}

bool FTLLRN_RigHierarchyPoseAdapter::ShrinkHierarchyStorage()
{
	if(UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		return Hierarchy->ShrinkElementStorage();
	}
	return false;
}

bool FTLLRN_RigHierarchyPoseAdapter::UpdateHierarchyStorage()
{
	if(UTLLRN_RigHierarchy* Hierarchy = GetHierarchy())
	{
		Hierarchy->UpdateElementStorage();
		return true;
	}
	return false;
}
