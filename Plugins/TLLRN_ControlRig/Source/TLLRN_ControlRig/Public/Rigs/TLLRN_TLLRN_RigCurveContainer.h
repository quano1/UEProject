// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Stats/StatsHierarchical.h"
#include "TLLRN_RigHierarchyDefines.h"
#include "TLLRN_RigHierarchyPose.h"
#include "TLLRN_TLLRN_RigCurveContainer.generated.h"

class UTLLRN_ControlRig;
class USkeleton;
struct FTLLRN_RigHierarchyContainer;

USTRUCT()
struct FTLLRN_RigCurve : public FTLLRN_RigElement
{
	GENERATED_BODY()

	FTLLRN_RigCurve()
		: FTLLRN_RigElement()
		, Value(0.f)
	{
	}
	virtual ~FTLLRN_RigCurve() {}

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = FTLLRN_RigElement)
	float Value;

	virtual ETLLRN_RigElementType GetElementType() const override
	{
		return ETLLRN_RigElementType::Curve;
	}
};

USTRUCT()
struct TLLRN_CONTROLRIG_API FTLLRN_TLLRN_RigCurveContainer
{
	GENERATED_BODY()

public:

	FTLLRN_TLLRN_RigCurveContainer();

	TArray<FTLLRN_RigCurve>::RangedForIteratorType      begin()       { return Curves.begin(); }
	TArray<FTLLRN_RigCurve>::RangedForConstIteratorType begin() const { return Curves.begin(); }
	TArray<FTLLRN_RigCurve>::RangedForIteratorType      end()         { return Curves.end();   }
	TArray<FTLLRN_RigCurve>::RangedForConstIteratorType end() const   { return Curves.end();   }

	FTLLRN_RigCurve& Add(const FName& InNewName, float InValue);

	// Pretty weird that this type is copy/move assignable (needed for USTRUCTs) but not copy/move constructible
	FTLLRN_TLLRN_RigCurveContainer(FTLLRN_TLLRN_RigCurveContainer&& InOther) = delete;
	FTLLRN_TLLRN_RigCurveContainer(const FTLLRN_TLLRN_RigCurveContainer& InOther) = delete;
	FTLLRN_TLLRN_RigCurveContainer& operator=(FTLLRN_TLLRN_RigCurveContainer&& InOther) = default;
	FTLLRN_TLLRN_RigCurveContainer& operator=(const FTLLRN_TLLRN_RigCurveContainer& InOther) = default;

private:
	UPROPERTY(EditAnywhere, Category = FTLLRN_TLLRN_RigCurveContainer)
	TArray<FTLLRN_RigCurve> Curves;

	friend struct FTLLRN_RigHierarchyContainer;
	friend struct FTLLRN_CachedTLLRN_RigElement;
	friend class UTLLRN_ControlTLLRN_RigHierarchyModifier;
};

