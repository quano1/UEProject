// Copyright Epic Games, Inc. All Rights Reserved.

#include "Units/Debug/TLLRN_RigUnit_ProfilingBracket.h"
#include "Units/TLLRN_RigUnitContext.h"
#include "TLLRN_ControlRigDefines.h"
#include "KismetAnimationLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_RigUnit_ProfilingBracket)

FTLLRN_RigUnit_StartProfilingTimer_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	UKismetAnimationLibrary::K2_StartProfilingTimer();
}

FTLLRN_RigUnit_EndProfilingTimer_Execute()
{
    DECLARE_SCOPE_HIERARCHICAL_COUNTER_RIGUNIT()
	if (!bIsInitialized)
	{
		AccumulatedTime = 0.f;
		MeasurementsLeft = FMath::Max(NumberOfMeasurements, 1);
		bIsInitialized = true;
	}

	float Delta = UKismetAnimationLibrary::K2_EndProfilingTimer(false);
	if(MeasurementsLeft > 0)
	{
		AccumulatedTime += Delta / float(FMath::Max(NumberOfMeasurements, 1));
		MeasurementsLeft--;

		if (MeasurementsLeft == 0)
		{
			if (Prefix.IsEmpty())
			{
				UE_TLLRN_CONTROLRIG_RIGUNIT_LOG_MESSAGE(TEXT("%0.3f ms (%d runs)."), AccumulatedTime, FMath::Max(NumberOfMeasurements, 1));
			}
			else
			{
				UE_TLLRN_CONTROLRIG_RIGUNIT_LOG_MESSAGE(TEXT("[%s] %0.3f ms (%d runs)."), *Prefix, AccumulatedTime, FMath::Max(NumberOfMeasurements, 1));
			}
		}
	}
}
