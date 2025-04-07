// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#if WITH_EDITOR
#include "PropertyPath.h"
#endif
#include "TLLRN_ControlRigDefines.h"
namespace UtilityHelpers
{
	template <typename Predicate>
	FName CreateUniqueName(const FName& InBaseName, Predicate IsUnique)
	{
		FName CurrentName = InBaseName;
		int32 CurrentIndex = 0;

		while (!IsUnique(CurrentName))
		{
			FString PossibleName = InBaseName.ToString() + TEXT("_") + FString::FromInt(CurrentIndex++);
			CurrentName = FName(*PossibleName);
		}

		return CurrentName;
	}

	template <typename Predicate>
	FTransform GetBaseTransformByMode(ETLLRN_TransformSpaceMode TLLRN_TransformSpaceMode, Predicate TransformGetter, const FTLLRN_RigElementKey& ParentKey, const FTLLRN_RigElementKey& BaseKey, const FTransform& BaseTransform)
	{
		switch (TLLRN_TransformSpaceMode)
		{
		case ETLLRN_TransformSpaceMode::LocalSpace:
		{
			return TransformGetter(ParentKey);
		}
		case ETLLRN_TransformSpaceMode::BaseSpace:
		{
			return BaseTransform;
		}
		case ETLLRN_TransformSpaceMode::BaseJoint:
		{
			return TransformGetter(BaseKey);
		}
		case ETLLRN_TransformSpaceMode::GlobalSpace:
		default:
		{
			return FTransform::Identity;
		}
		}
	}
}
