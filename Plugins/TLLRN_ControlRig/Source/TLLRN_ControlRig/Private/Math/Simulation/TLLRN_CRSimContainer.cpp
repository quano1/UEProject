// Copyright Epic Games, Inc. All Rights Reserved.

#include "Math/Simulation/TLLRN_CRSimContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TLLRN_CRSimContainer)

void FTLLRN_CRSimContainer::Reset()
{
	AccumulatedTime = TimeLeftForStep = 0.f;
}

void FTLLRN_CRSimContainer::ResetTime()
{
	AccumulatedTime = TimeLeftForStep = 0.f;
}

void FTLLRN_CRSimContainer::StepVerlet(float InDeltaTime, float InBlend)
{
	TimeLeftForStep -= InDeltaTime;

	if (TimeLeftForStep >= 0.f)
	{
		return;
	}

	while (TimeLeftForStep < 0.f)
	{
		TimeLeftForStep += TimeStep;
		AccumulatedTime += TimeStep;
		CachePreviousStep();
		IntegrateVerlet(InBlend);
	}
}

void FTLLRN_CRSimContainer::StepSemiExplicitEuler(float InDeltaTime)
{
	TimeLeftForStep -= InDeltaTime;

	if (TimeLeftForStep >= 0.f)
	{
		return;
	}

	while (TimeLeftForStep < 0.f)
	{
		TimeLeftForStep += TimeStep;
		AccumulatedTime += TimeStep;
		CachePreviousStep();
		IntegrateSemiExplicitEuler();
	}
}

