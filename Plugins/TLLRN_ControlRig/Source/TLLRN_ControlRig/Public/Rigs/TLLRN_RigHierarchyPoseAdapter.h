// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TLLRN_RigHierarchyElements.h"

class UTLLRN_RigHierarchy;

class TLLRN_CONTROLRIG_API FTLLRN_RigHierarchyPoseAdapter : public TSharedFromThis<FTLLRN_RigHierarchyPoseAdapter>
{
public:

	virtual ~FTLLRN_RigHierarchyPoseAdapter()
	{
	}

protected:

	UTLLRN_RigHierarchy* GetHierarchy() const;

	virtual void PostLinked(UTLLRN_RigHierarchy* InHierarchy);
	virtual void PreUnlinked(UTLLRN_RigHierarchy* InHierarchy);
	virtual void PostUnlinked(UTLLRN_RigHierarchy* InHierarchy);

	TTuple<FTLLRN_RigComputedTransform*, FTLLRN_RigTransformDirtyState*> GetElementTransformStorage(const FTLLRN_RigElementKeyAndIndex& InKeyAndIndex, ETLLRN_RigTransformType::Type InTransformType, ETLLRN_RigTransformStorageType::Type InStorageType) const;

	bool RelinkTransformStorage(const FTLLRN_RigElementKeyAndIndex& InKeyAndIndex, ETLLRN_RigTransformType::Type InTransformType, ETLLRN_RigTransformStorageType::Type InStorageType, FTransform* InTransformStorage, bool* InDirtyFlagStorage);
	bool RestoreTransformStorage(const FTLLRN_RigElementKeyAndIndex& InKeyAndIndex, ETLLRN_RigTransformType::Type InTransformType, ETLLRN_RigTransformStorageType::Type InStorageType, bool bUpdateElementStorage);
	bool RelinkTransformStorage(const TArrayView<TTuple<FTLLRN_RigElementKeyAndIndex,ETLLRN_RigTransformType::Type,ETLLRN_RigTransformStorageType::Type,FTransform*,bool*>>& InData);
	bool RestoreTransformStorage(const TArrayView<TTuple<FTLLRN_RigElementKeyAndIndex,ETLLRN_RigTransformType::Type,ETLLRN_RigTransformStorageType::Type>>& InData, bool bUpdateElementStorage);

	bool RelinkCurveStorage(const FTLLRN_RigElementKeyAndIndex& InKeyAndIndex, float* InCurveStorage);
	bool RestoreCurveStorage(const FTLLRN_RigElementKeyAndIndex& InKeyAndIndex, bool bUpdateElementStorage);
	bool RelinkCurveStorage(const TArrayView<TTuple<FTLLRN_RigElementKeyAndIndex,float*>>& InData);
	bool RestoreCurveStorage(const TArrayView<FTLLRN_RigElementKeyAndIndex>& InData, bool bUpdateElementStorage);

	bool SortHierarchyStorage();
	bool ShrinkHierarchyStorage();
	bool UpdateHierarchyStorage();

	TWeakObjectPtr<UTLLRN_RigHierarchy> WeakHierarchy;

	friend class UTLLRN_RigHierarchy;
	friend class UTLLRN_RigHierarchyController;
	friend class FTLLRN_ControlTLLRN_RigHierarchyPoseAdapter;
};
